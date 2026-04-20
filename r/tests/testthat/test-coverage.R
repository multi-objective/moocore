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

}) # withr::with_output_sink()
