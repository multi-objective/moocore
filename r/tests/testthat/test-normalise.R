source("helper-common.R")

test_that("normalise", {
  my.2d.matrix <- function(...) matrix(c(...), ncol = 2, byrow=FALSE)
  x = my.2d.matrix(0, 0.5, 1, 0, 0.5, 1)

  expect_equal(normalise(x), my.2d.matrix(1, 1.5, 2, 1, 1.5, 2))

  expect_equal(normalise(x, maximise = c(FALSE,TRUE)),
               my.2d.matrix(1, 1.5, 2, 2, 1.5, 1))

  expect_equal(normalise(x, to_range = c(0, 1), maximise = c(FALSE,TRUE)),
               my.2d.matrix(0, 0.5, 1, 1, 0.5, 0))

  expect_equal(normalise(my.2d.matrix(1,1,2,2)), my.2d.matrix(1,1,1,1))

  df <- as.data.frame(x)
  expect_equal(normalise(df),
               as.matrix(data.frame(V1=c(1,1.5,2), V2=c(1,1.5,2))))
})
