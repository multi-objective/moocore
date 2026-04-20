test_that("bug 27", {
  x <- matrix(c(0.5,0.6,0.3,0.1,0.0,0.9,0.0), ncol=1L)
  expect_equal(is_nondominated(x),
      c(FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE))
  expect_equal(is_nondominated(x, keep_weakly=TRUE),
    c(FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE))
  expect_equal(is_nondominated(x, maximise=TRUE),
    c(FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE))
  expect_equal(is_nondominated(x, keep_weakly=TRUE, maximise=TRUE),
    c(FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE))
  expect_true(any_dominated(x))
  expect_false(any_dominated(matrix(rep(5,5), ncol=1L), keep_weakly=TRUE))

})

test_that("is_nondominated 4D", {
  set.seed(42)
  pts <- matrix(runif(100), ncol = 4L)  # 25 points in 4D

  fd <- filter_dominated(pts)
  expect_true(nrow(fd) >= 1L)
  nd <- is_nondominated(fd)
  expect_true(is.logical(nd))
  expect_equal(length(nd), nrow(fd))
  # The non-dominated points should not dominate each other
  expect_true(all(nd))
  expect_false(any_dominated(fd))
})

test_that("is_nondominated 5D", {
  set.seed(123)
  pts <- matrix(runif(50), ncol = 5)  # 10 points in 5D

  fd <- filter_dominated(pts)
  expect_true(nrow(fd) >= 1L)
  nd <- is_nondominated(fd)
  expect_true(is.logical(nd))
  expect_equal(length(nd), nrow(fd))
  # The non-dominated points should not dominate each other
  expect_true(all(nd))
  expect_false(any_dominated(fd))
})
