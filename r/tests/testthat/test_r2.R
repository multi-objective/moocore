test_that("r2_exact", {
  # perfect approximation of ideal ref yields 0.0
  expect_equal(r2_exact(matrix(c(0, 0), ncol = 2), reference = c(0, 0)), 0.0)

  # [1,1] in normalized objective space should yield r2 = 0.75
  expect_equal(r2_exact(matrix(c(2, 2), ncol = 2), reference = c(1, 1)), 0.75)

  # [[1,0],[0,1]] should yield r2 = 0.25
  expect_equal(r2_exact(matrix(c(0, 1, 1, 0), ncol = 2, byrow = TRUE), reference = c(0, 0)), 0.25)

  x <- matrix(c(0, 1, 0.2, 0.8, 0.4, 0.6, 0.6, 0.4, 0.8, 0.2, 1, 0), ncol = 2, byrow = TRUE)
  expect_equal(r2_exact(x, reference = c(0, 0)), 0.1833333333333333, tolerance = 1e-10)

  # a closely sampled linear front should yield a value close to 1/6
  xvals <- seq(0, 1, length.out = 1000001)
  pf <- cbind(xvals, 1 - xvals)
  expect_equal(r2_exact(pf, reference = c(0, 0)), 1 / 6, tolerance = 1e-6)
})

test_that("r2_exact from doc", {
  dat <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol = 2, byrow = TRUE)
  expect_equal(r2_exact(dat, reference = c(0, 0)), 2.5941919191919194, tolerance = 1e-9)

  expect_equal(r2_exact(dat, reference = c(10, 10), maximise = TRUE), 2.5196969696969695, tolerance = 1e-9)

  extdata_path <- system.file(package = "moocore", "extdata")
  dat <- read_datasets(file.path(extdata_path, "example1_dat"))[, 1:2]
  r2_val <- r2_exact(dat, reference = c(0, 0))
  dat_nondom <- filter_dominated(dat)
  r2_val_nondom <- r2_exact(dat_nondom, reference = c(0, 0))
  # Dominated points are ignored, so both should give the same value
  expect_equal(r2_val, r2_val_nondom)
})
