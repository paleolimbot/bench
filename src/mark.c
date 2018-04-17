#include <Rinternals.h>
#include <stdio.h>
#include <stdlib.h>
#include "nanotime.h"
#include <unistd.h>

SEXP mark_(SEXP expr, SEXP env, SEXP min_time, SEXP num_iterations, SEXP logfile) {
  R_xlen_t n = INTEGER(num_iterations)[0];
  double min = REAL(min_time)[0];

  const char* log = CHAR(STRING_ELT(logfile, 0));
  SEXP out = PROTECT(Rf_allocVector(VECSXP, 2));
  SET_VECTOR_ELT(out, 0, Rf_allocVector(REALSXP, n));
  SET_VECTOR_ELT(out, 1, Rf_allocVector(STRSXP, n));

  long double begin;
  if (NANO_UNEXPECTED(nano_time(&begin))) {
    Rf_error("Failed to get begin time");
  }
  R_xlen_t i = 0;
  for (; i < n; ++i) {
    long double start, end;
    if (NANO_UNEXPECTED(nano_time(&start))) {
      Rf_error("Failed to get start time iteration: %i", i);
    }

    freopen(log, "w", stderr);

    // Actually evaluate the R code
    Rf_eval(expr, env);

    FILE* fp = fopen(log, "r");
    char* buffer = NULL;
    size_t len;
    ssize_t bytes_read = getdelim( &buffer, &len, '\0', fp);
    if ( bytes_read != -1) {
      SET_STRING_ELT(VECTOR_ELT(out, 1), i, Rf_mkChar(buffer));
      free(buffer);
    }
    fclose(fp);

    if (NANO_UNEXPECTED(nano_time(&end))) {
      Rf_error("Failed to get end time iteration: %i", i);
    }

    // If we are over our time threshold for this expression break
    if (min != 0 && end - begin > min) { break; }

    REAL(VECTOR_ELT(out, 0))[i] = end - start;
  }

  SET_VECTOR_ELT(out, 0, Rf_lengthgets(VECTOR_ELT(out, 0), i));
  SET_VECTOR_ELT(out, 1, Rf_lengthgets(VECTOR_ELT(out, 1), i));

  freopen("/dev/tty", "a", stderr);

  UNPROTECT(1);

  return out;
}

static const R_CallMethodDef CallEntries[] = {
    {"mark_", (DL_FUNC) &mark_, 5},
    {NULL, NULL, 0}
};

void R_init_bench(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
