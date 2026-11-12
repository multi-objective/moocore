source("helper-common.R")

withr::with_output_sink("test-whv_hype.Rout", {

find_minmax <- function(...)
{
  args <- list(...)
  lower <- list()
  upper <- list()
  for (x in args) {
    stopifnot(is.numeric(x))
    minmax <- apply(x[,1:2], 2L, range)
    lower <- c(lower, list(minmax[1L, , drop = FALSE]))
    upper <- c(upper, list(minmax[2L, , drop = FALSE]))
  }
  lower <- apply(do.call("rbind", lower), 2L, min)
  upper <- apply(do.call("rbind", upper), 2L, min)
  list(lower = lower, upper = upper)
}

test_that("whv_hype", {

  x <- read_extdata("wrots_l10w100_dat")
  y <- read_extdata("wrots_l100w10_dat")

  r <- find_minmax(x, y)
  ideal <- r$lower
  ref <- 1.1 * r$upper
  goal <- colMeans(x[,1:2])

  x_list <- split.data.frame(x[,1:2], x[,3])
  y_list <- split.data.frame(y[,1:2], y[,3])
  whv_x <- whv_hype(x_list[[1]], reference = ref, ideal = ideal, seed = 42)
  expect_equal(whv_x, 2480047738211, tolerance=1e-5)

  whv_x <- whv_hype(x_list[[1]], reference = ref, ideal = ideal,
                    dist = "point", mu = goal, seed = 42)
  expect_equal(whv_x, 1496335657875, tolerance=1e-5)

  whv_x <- whv_hype(x_list[[1]], reference = ref, ideal = ideal,
                    dist = "exponential", mu=0.2, seed = 42)
  expect_equal(whv_x, 1902086184412, tolerance=1e-5)
})


test_that("whv_hype mixed min/max", {
  expect_equal(whv_hype(matrix(-2, ncol = 2), reference = -4, ideal = -1, seed = 42, maximise=TRUE),
    3.9861)
  expect_equal(whv_hype(matrix(c(-2, 2), ncol = 2), reference = c(-4, 4), ideal = c(-1, 1), seed = 42, maximise=c(TRUE,FALSE)),
    3.9861)

})

test_that("whv_hype requires mu for non-uniform dist", {
  expect_error(
    whv_hype(matrix(2, ncol = 2), reference = 4, ideal = 1, dist = "point"),
    "mu.*required"
  )
})



}) # withr::with_output_sink
