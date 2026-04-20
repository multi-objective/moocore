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

}) # withr::with_output_sink()
