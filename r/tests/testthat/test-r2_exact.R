
test_that("r2_exact mixed maximise", {
  dat <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol = 2, byrow = TRUE)
  dat[, 1] <- -dat[, 1]
  expect_equal(tolerance = 1e-09,
    r2_exact(dat, reference=c(0,0), maximise=c(TRUE,FALSE)), 2.59419191919192)
  expect_equal(tolerance = 1e-09,
    r2_exact(dat, reference = c(-10, 10), maximise = c(FALSE, TRUE)), 2.51969696969697)
})
