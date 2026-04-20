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

}) # withr::with_output_sink()
