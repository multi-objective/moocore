source("helper-common.R")

withr::with_output_sink("test-coverage.Rout", {

# -----------------------------------------------------------------------
# hv_contributions with 3D data (exercises hvc3d.c)
# -----------------------------------------------------------------------
test_that("hv_contributions 3D", {
  hv_contributions_slow_3d <- function(dataset, reference) {
    nondom <- is_nondominated(dataset, keep_weakly = TRUE)
    hvc <- numeric(nrow(dataset))
    ds <- dataset[nondom, , drop = FALSE]
    hvc[nondom] <- hypervolume(ds, reference) -
      sapply(seq_len(nrow(ds)), function(i) hypervolume(ds[-i, , drop = FALSE], reference = reference))
    hvc
  }
  set.seed(42)
  pts <- matrix(runif(30), ncol = 3)
  ref <- c(2, 2, 2)
  hvc_fast <- hv_contributions(pts, reference = ref)
  hvc_slow <- hv_contributions_slow_3d(pts, ref)
  expect_equal(hvc_fast, hvc_slow, tolerance = 1e-10)
})

# -----------------------------------------------------------------------
# is_nondominated / filter_dominated with 4+ objectives (exercises nondominated_kung.h)
# -----------------------------------------------------------------------
test_that("is_nondominated 4D", {
  set.seed(42)
  pts <- matrix(runif(100), ncol = 4)  # 25 points in 4D
  nd <- is_nondominated(pts)
  expect_true(is.logical(nd))
  expect_equal(length(nd), nrow(pts))
  # The non-dominated points should not dominate each other
  nd_pts <- pts[nd, , drop = FALSE]
  if (nrow(nd_pts) > 1) {
    expect_true(all(is_nondominated(nd_pts)))
  }
})

test_that("filter_dominated 5D", {
  set.seed(123)
  pts <- matrix(runif(50), ncol = 5)  # 10 points in 5D
  fd <- filter_dominated(pts)
  expect_true(nrow(fd) >= 1)
  expect_true(all(is_nondominated(fd)))
})

# -----------------------------------------------------------------------
# whv_hype with maximise variants
# -----------------------------------------------------------------------
test_that("whv_hype maximise=TRUE", {
  x <- matrix(c(3, 1), ncol = 2)
  # When all maximise, the problem is flipped
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
# vorob_t and vorob_dev with maximise
# -----------------------------------------------------------------------
test_that("vorob_t maximise=TRUE", {
  data(CPFs)
  # Negate data to simulate maximisation
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
# eafdiff with maximise variants
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
# utils.R error paths
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
# rbind_datasets error paths
# -----------------------------------------------------------------------
test_that("rbind_datasets errors", {
  x_small <- data.frame(f1 = 1:3, f2 = 4:6)
  y <- data.frame(f1 = 7:9, f2 = 10:12, set = 1:3)
  expect_error(rbind_datasets(x_small, x_small))

  # Sets not starting at 1
  x_bad <- data.frame(f1 = 1:3, f2 = 4:6, set = 2:4)
  expect_error(rbind_datasets(x_bad, y))
})

# -----------------------------------------------------------------------
# r2_exact with mixed maximise
# -----------------------------------------------------------------------
test_that("r2_exact mixed maximise", {
  dat <- matrix(c(5, 5, 4, 6, 2, 7, 7, 4), ncol = 2, byrow = TRUE)
  result <- r2_exact(dat, reference = c(0, 10), maximise = c(FALSE, TRUE))
  expect_true(is.numeric(result))
  expect_true(is.finite(result))
})

# -----------------------------------------------------------------------
# epsilon with mixed maximise
# -----------------------------------------------------------------------
test_that("epsilon_additive mixed maximise", {
  A <- matrix(c(4, 2, 3, 3, 2, 4), ncol = 2, byrow = TRUE)
  ref <- matrix(c(10, 1, 6, 1, 2, 2, 1, 6, 1, 10), ncol = 2, byrow = TRUE)
  result <- epsilon_additive(A, ref, maximise = c(TRUE, FALSE))
  expect_true(is.numeric(result))
  expect_true(is.finite(result))
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

}) # withr::with_output_sink()
