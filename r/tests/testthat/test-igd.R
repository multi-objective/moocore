# Example 4 from Ishibuchi et al. (2015)
ref = matrix(c(10, 0, 6, 1, 2, 2, 1, 6, 0, 10), ncol = 2L, byrow = TRUE)
A = matrix(c(4, 2, 3, 3, 2, 4), ncol = 2L, byrow = TRUE)
B = matrix(c(8, 2, 4, 4, 2, 8), ncol = 2L, byrow = TRUE)

test_that("igd", {
  expect_equal(igd(A, ref), 3.707092031609239)
  expect_equal(igd(B, ref), 2.59148346584763)
})

test_that("igd plus", {
  expect_equal(igd_plus(A, ref), 1.482842712474619)
  expect_equal(igd_plus(B, ref), 2.260112615949154)

})
test_that("avg_hausdorff_dist", {
  expect_equal(avg_hausdorff_dist(A, ref), 3.707092031609239)
  expect_equal(avg_hausdorff_dist(B, ref), 2.59148346584763)
})
