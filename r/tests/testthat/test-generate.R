
test_that("generate_ndset", {
  n <- 10L
  one <- replicate(n,1)
  for (dim in seq(5,10)) {
    points <- generate_ndset(n, dim, "simplex")
    expect_equal(rowSums(points), one)
    points <- generate_ndset(n, dim, "concave-sphere")
    expect_equal(rowSums(points**2), one)
    points <- generate_ndset(n, dim, "convex-simplex")
    expect_equal(rowSums(sqrt(points)), one)
  }
})
