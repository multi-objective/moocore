test_that("epsilon", {
  ref = matrix(c(10, 1, 6, 1, 2, 2, 1, 6, 1, 10), ncol = 2L, byrow = TRUE)
  A = matrix(c(4, 2, 3, 3, 2, 4), ncol = 2L, byrow = TRUE)
  expect_equal(epsilon_additive(A, ref), 1.0)
  expect_equal(epsilon_mult(A, ref), 2.0)
  expect_equal(epsilon_mult(A, ref, maximise=TRUE), 2.5)
  expect_equal(epsilon_additive(A, ref, maximise=TRUE), 6.0)
})
