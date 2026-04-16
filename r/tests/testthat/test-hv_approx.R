test_that("hv_approx errors", {
  x <- matrix(c(5, 5, 4, 6), ncol = 2, byrow = TRUE)
  expect_error(hv_approx(x, ref = NULL), "a numerical reference vector must be provided")
  expect_error(hv_approx(x, ref = 10, method = "None"), "'arg' should be one of ")
  expect_equal(hv_approx(x, ref = 1), 0)
})

for (dim in seq(3L, 10L)) {
  test_that(paste0("hv_approx dim=", dim), {
    x <- matrix(replicate(dim, 0.5), ncol=dim)
    ref <- 1.0
    true_hv <- hypervolume(x, ref=ref)

    ## Precision goes down significantly with higher dimensions.
    signif <- if (dim < 4) 3 else 2
    expect_equal(hv_approx(x, ref=ref, method="DZ2019-MC", seed=42), true_hv,
      tolerance = 10**-signif, info = paste0("dim=", dim, ", signif=", signif,
        " error=", log10(abs(true_hv - appr_hv) / true_hv)))

    signif <- if (dim < 8) 4 else if (dim < 10) 3 else 2
    expect_equal(hv_approx(x, ref=ref, method="DZ2019-HW"), true_hv,
      tolerance = 10**-signif, info = paste0("dim=", dim, ", signif=", signif,
        " error=", log10(abs(true_hv - appr_hv) / true_hv)))

    # method="Rphi-FWE+" is the default
    signif <- if (dim < 6) 4 else if (dim < 8) 3 else 2
    expect_equal(hv_approx(x, ref=ref), true_hv,
      tolerance = 10**-signif, info = paste0("dim=", dim, ", signif=", signif,
        " error=", log10(abs(true_hv - appr_hv) / true_hv)))
  })
}
