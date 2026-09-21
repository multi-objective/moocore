
test_that("generate_ndset", {
  n <- 10L
  one <- replicate(n,1)
  for (dim in seq(5,10)) {
    points <- generate_ndset(n, dim, "simplex")
    expect_false(any_dominated(points))
    expect_equal(rowSums(points), one)
    points <- generate_ndset(n, dim, "concave-sphere")
    expect_false(any_dominated(points))
    expect_equal(rowSums(points**2), one)
    points <- generate_ndset(n, dim, "convex-simplex")
    expect_false(any_dominated(points))
    expect_equal(rowSums(sqrt(points)), one)
    points <- generate_ndset(n, dim, "cliff-concave")
    expect_false(any_dominated(points))
    expect_equal(rowSums(points[, 1:2]^2), one)
    expect_true(all(points[, 3:dim] >= 0 & points[, 3:dim] <= 1))
    points <- generate_ndset(n, dim, "cliff-convex")
    expect_false(any_dominated(points))
    expect_equal(rowSums((1 - points[, 1:2])^2), one)
    expect_true(all(points[, 3:dim] >= 0 & points[, 3:dim] <= 1))
  }

  expect_error(
    generate_ndset(n, 2L, "cliff-concave"),
    "requires at least 3 dimensions"
  )
  expect_error(
    generate_ndset(n, 2L, "cliff-convex"),
    "requires at least 3 dimensions"
  )
})
