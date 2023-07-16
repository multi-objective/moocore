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
  set.seed(12345)
  whv_x <- whv_hype(x_list[[1]], reference = ref, ideal = ideal)
  expect_equal(whv_x, 2480979524388, tolerance=10)

  whv_x <- whv_hype(x_list[[1]], reference = ref, ideal = ideal,
                    dist = "point", mu = goal)
  expect_equal(whv_x, 1496335657875, tolerance=10)

  whv_x <- whv_hype(x_list[[1]], reference = ref, ideal = ideal,
                    dist = "exponential", mu=0.2)
  expect_equal(whv_x, 1903385037871, tolerance=10)

})

})
