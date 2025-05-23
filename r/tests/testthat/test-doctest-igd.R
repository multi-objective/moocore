# Generated by doctest: do not edit by hand
# Please edit file in R/igd.R

test_that("Doctest: igd", {
  # Created from @doctest for `igd`
  # Source file: R/igd.R
  # Source line: 74
  extdata_path <- system.file(package = "moocore", "extdata")
  path.A1 <- file.path(extdata_path, "ALG_1_dat.xz")
  path.A2 <- file.path(extdata_path, "ALG_2_dat.xz")
  A1 <- read_datasets(path.A1)[, 1:2]
  A2 <- read_datasets(path.A2)[, 1:2]
  ref <- filter_dominated(rbind(A1, A2))
  expect_equal(igd(A1, ref), 91888189)
  expect_equal(igd(A2, ref), 11351992)
  expect_equal(igd_plus(A1, ref), 82695357)
  expect_equal(igd_plus(A2, ref), 10698269.3)
  expect_equal(avg_hausdorff_dist(A1, ref), 268547627)
  expect_equal(avg_hausdorff_dist(A2, ref), 352613092)
})

