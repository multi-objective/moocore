
test_that("generate_ndset", {
  n <- 10L
  one <- rep(1, n)
  expect_ndset <- function(points) {
    expect_false(any_dominated(points))
    expect_true(all(points >= 0 & points <= 1))
  }
  expect_cliff_pair_norms <- function(points, complement = FALSE) {
    for (i in seq_len(ncol(points) %/% 2L)) {
      cols <- (2L * i - 1L):(2L * i)
      pair <- points[, cols]
      if (complement) {
        pair <- 1 - pair
      }
      expect_equal(rowSums(pair^2), rep(1, nrow(points)))
    }
  }

  for (dim in seq(2, 8)) {
    seed <- sample.int(2^31, size=1L)
    points <- generate_ndset(n, dim, "simplex", seed=seed)
    expect_equal(points, generate_ndset(n, dim, "L", seed=seed))
    expect_equal(points, generate_ndset(n, dim, "linear", seed=seed))
    expect_ndset(points)
    expect_equal(rowSums(points), one)
    points <- generate_ndset(n, dim, "inverted-simplex", seed=seed)
    expect_equal(points, generate_ndset(n, dim, "inverted-linear", seed=seed))
    expect_ndset(points)
    expect_equal(rowSums(1 - points), one)
    points <- generate_ndset(n, dim, "concave-sphere", seed=seed)
    expect_equal(points, generate_ndset(n, dim, "sphere", seed=seed))
    expect_equal(points, generate_ndset(n, dim, "C", seed=seed))
    expect_ndset(points)
    expect_equal(rowSums(points**2), one)
    points <- generate_ndset(n, dim, "convex-sphere", seed=seed)
    expect_equal(points, generate_ndset(n, dim, "X", seed=seed))
    expect_ndset(points)
    expect_equal(rowSums((1 - points)**2), one)
    points <- generate_ndset(n, dim, "concave-simplex")
    expect_ndset(points)
    expect_equal(rowSums(sqrt(1 - points)), one)
    points <- generate_ndset(n, dim, "convex-simplex")
    expect_ndset(points)
    expect_equal(rowSums(sqrt(points)), one)
    if (dim >= 3) {
      points <- generate_ndset(n, dim, "concave-cliff")
      expect_ndset(points)
      expect_cliff_pair_norms(points)
      points <- generate_ndset(n, dim, "convex-cliff")
      expect_ndset(points)
      expect_cliff_pair_norms(points, complement = TRUE)
    }
  }

  expect_error(generate_ndset(n, 2L, "concave-cliff"),
    "requires at least 3 dimensions")
  expect_error(generate_ndset(n, 2L, "convex-cliff"),
    "requires at least 3 dimensions")
})
