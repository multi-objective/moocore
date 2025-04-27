test_that("vorob", {
  # FIXME: What is this test exactly testing?
  test_data <- expand.grid(seq(0, 1, length.out = 51),
                           seq(0, 1, length.out = 51))
  test_data <- as.matrix(cbind(test_data, nrun = 1:nrow(test_data)))

  # Average hypervolume is known to be 0.25
  # avg_hyp <- mean(sapply(split.data.frame(test_data[,1:2], test_data[,3]),
  #                        hypervolume, reference = c(1, 1)))
  expect_equal(hypervolume(vorob_t(x = test_data, reference = c(1, 1))$ve, reference = c(1, 1)), 0.25, tolerance = 1e-1)
  expect_equal(vorob_dev(x = test_data, ve = vorob_t(test_data, reference = c(1,1))$ve, reference = c(1, 1)), 0.218, tolerance = 1e-1)

})
