source("helper-common.R")

withr::with_output_sink("test-coverage.Rout", {

# -----------------------------------------------------------------------
# eafdiff polygon computation (exercises eaf_compute_area in c/eaf.c lines 421-779)
# -----------------------------------------------------------------------
test_that("compute_eafdiff_polygon", {
  A1 <- read_datasets(text = "3 2\n2 3\n\n2.5 1\n1 2\n")
  A2 <- read_datasets(text = "4 2.5\n3 3\n2.5 3.5\n\n3 3\n2.5 3.5\n")
  # compute_eafdiff_polygon calls eaf_compute_area (alias eaf_compute_polygon)
  # in c/eaf.c, covering lines 421-779 which are otherwise unreachable.
  result <- compute_eafdiff_polygon(A1[, 1:2], A2[, 1:2], A1[, 3], A2[, 3],
                                    intervals = 5L)
  expect_true(is.list(result))
  expect_equal(length(result), 2L)
})

# -----------------------------------------------------------------------
# whv_hype with maximise variants (r/R/whv.R from 61%)
# -----------------------------------------------------------------------
test_that("whv_hype maximise=TRUE", {
  x <- matrix(c(3, 1), ncol = 2)
  result <- whv_hype(x, reference = 0, ideal = 4, seed = 42, maximise = TRUE)
  expect_true(is.numeric(result))
  expect_true(result > 0)
})

test_that("whv_hype maximise=c(TRUE,FALSE)", {
  x <- matrix(c(3, 1), ncol = 2)
  result <- whv_hype(x, reference = c(0, 4), ideal = c(4, 0), seed = 42,
                     maximise = c(TRUE, FALSE))
  expect_true(is.numeric(result))
  expect_true(result > 0)
})

test_that("whv_hype requires mu for non-uniform dist", {
  expect_error(
    whv_hype(matrix(2, ncol = 2), reference = 4, ideal = 1, dist = "point"),
    "mu.*required"
  )
})

# -----------------------------------------------------------------------
# total_whv_rect error paths
# -----------------------------------------------------------------------
test_that("total_whv_rect ideal length mismatch", {
  rectangles <- as.matrix(read.table(header = FALSE, text = "
    1.0  3.0  2.0  Inf    1
    2.0  3.5  2.5  Inf    2
  "))
  expect_error(
    total_whv_rect(matrix(2, ncol = 2), rectangles, reference = 6,
                   ideal = c(1, 1, 1)),
    "ideal.*same length"
  )
})

test_that("total_whv_rect scalefactor validation", {
  rectangles <- as.matrix(read.table(header = FALSE, text = "
    1.0  3.0  2.0  Inf    1
  "))
  expect_error(
    total_whv_rect(matrix(2, ncol = 2), rectangles, reference = 6,
                   ideal = c(1, 1), scalefactor = 0),
    "scalefactor"
  )
})

# -----------------------------------------------------------------------
# utils.R error paths (from 68%)
# -----------------------------------------------------------------------
test_that("as_double_matrix errors", {
  expect_error(as_double_matrix(1:5), "must be a data.frame or a matrix")
  expect_error(as_double_matrix(matrix(nrow = 0, ncol = 3)), "not enough points")
  expect_error(as_double_matrix(matrix(1, nrow = 1, ncol = 1)), "at least 2 columns")
  expect_error(as_double_matrix(matrix("a", nrow = 1, ncol = 2)), "must be numeric")
})

test_that("transform_maximise length mismatch", {
  x <- matrix(1:6, ncol = 3)
  expect_error(transform_maximise(x, c(TRUE, FALSE)), "length of maximise")
})

# -----------------------------------------------------------------------
# eafdiff with maximise variants (r/R/eafdiff.R from 74%)
# -----------------------------------------------------------------------
test_that("eafdiff maximise=TRUE", {
  A1 <- read_datasets(text = "3 2\n2 3\n\n2.5 1\n1 2\n")
  A2 <- read_datasets(text = "4 2.5\n3 3\n2.5 3.5\n\n3 3\n2.5 3.5\n")
  d <- eafdiff(A1, A2, maximise = TRUE)
  expect_true(is.matrix(d))
  expect_true(ncol(d) == 3)
})

test_that("eafdiff rectangles with maximise vector", {
  A1 <- read_datasets(text = "3 2\n2 3\n\n2.5 1\n1 2\n")
  A2 <- read_datasets(text = "4 2.5\n3 3\n2.5 3.5\n\n3 3\n2.5 3.5\n")
  d <- eafdiff(A1, A2, rectangles = TRUE, maximise = c(TRUE, FALSE))
  expect_true(is.matrix(d))
  expect_true(ncol(d) == 5)
})

# -----------------------------------------------------------------------
# vorob_t and vorob_dev with maximise (r/R/vorob.R from 77%)
# -----------------------------------------------------------------------
test_that("vorob_t maximise=TRUE", {
  data(CPFs)
  CPFs_neg <- CPFs
  CPFs_neg[, 1:2] <- -CPFs_neg[, 1:2]
  res <- vorob_t(CPFs_neg, reference = c(-2, -200), maximise = TRUE)
  expect_true(is.numeric(res$threshold))
  expect_true(is.matrix(res$ve))
})

test_that("vorob_dev maximise=TRUE", {
  data(CPFs)
  CPFs_neg <- CPFs
  CPFs_neg[, 1:2] <- -CPFs_neg[, 1:2]
  res <- vorob_t(CPFs_neg, reference = c(-2, -200), maximise = TRUE)
  vd <- vorob_dev(CPFs_neg, ve = res$ve, reference = c(-2, -200), maximise = TRUE)
  expect_true(is.numeric(vd))
})

# -----------------------------------------------------------------------
# rbind_datasets error paths (r/R/rbind_datasets.R from 77%)
# -----------------------------------------------------------------------
test_that("rbind_datasets errors", {
  x_small <- data.frame(f1 = 1:3, f2 = 4:6)
  y <- data.frame(f1 = 7:9, f2 = 10:12, set = 1:3)
  expect_error(rbind_datasets(x_small, x_small))

  x_bad <- data.frame(f1 = 1:3, f2 = 4:6, set = 2:4)
  expect_error(rbind_datasets(x_bad, y))
})

# -----------------------------------------------------------------------
# r2_exact with mixed maximise (r/R/r2.R from 78%)
# -----------------------------------------------------------------------
test_that("r2_exact mixed maximise", {
  dat <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol = 2, byrow = TRUE)
  result <- r2_exact(dat, reference = c(0, 10), maximise = c(FALSE, TRUE))
  expect_true(is.numeric(result))
  expect_true(is.finite(result))
})

# -----------------------------------------------------------------------
# epsilon with mixed maximise (r/R/epsilon.R from 84%)
# -----------------------------------------------------------------------
test_that("epsilon_additive mixed maximise", {
  A <- matrix(c(4, 2, 3, 3, 2, 4), ncol = 2, byrow = TRUE)
  ref <- matrix(c(10, 1, 6, 1, 2, 2, 1, 6, 1, 10), ncol = 2, byrow = TRUE)
  result <- epsilon_additive(A, ref, maximise = c(TRUE, FALSE))
  expect_true(is.numeric(result))
  expect_true(is.finite(result))
})

}) # withr::with_output_sink()
