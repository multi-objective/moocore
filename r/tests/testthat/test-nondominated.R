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
