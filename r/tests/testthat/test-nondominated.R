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

test_that("any_dominated 3D dominated", {
  # exercises find_dominated_3d_ (nondominated.h lines 449, 480)
  pts <- matrix(c(1,1,1, 0,0,0, 2,0,2, 0,2,0), nrow = 4L, ncol = 3L, byrow = TRUE)
  expect_true(any_dominated(pts))
  expect_false(any_dominated(filter_dominated(pts)))

  # 3D with a maximised objective: force_agree_minimize allocates a copy so
  # the free() at nondominated.h line 665 is exercised.
  pts_max <- matrix(c(1,1,1, 2,2,2, 0,3,0), nrow = 3L, ncol = 3L, byrow = TRUE)
  expect_true(any_dominated(pts_max, maximise = c(FALSE, FALSE, TRUE)))
  expect_false(any_dominated(filter_dominated(pts_max, maximise = c(FALSE, FALSE, TRUE)),
                             maximise = c(FALSE, FALSE, TRUE)))
})

test_that("any_dominated 4D agree cases", {
  # 4D mixed objectives (agree = AGREE_NONE):
  # exercises nondominated.h case AGREE_NONE (lines 624, 627)
  pts_mixed <- matrix(c(1,1,1,1, 2,2,0,0, 0,0,2,2), nrow = 3L, ncol = 4L, byrow = TRUE)
  expect_true(any_dominated(pts_mixed, maximise = c(FALSE, FALSE, TRUE, TRUE)))
  expect_false(any_dominated(filter_dominated(pts_mixed, maximise = c(FALSE, FALSE, TRUE, TRUE)),
                             maximise = c(FALSE, FALSE, TRUE, TRUE)))

  # 4D all-maximise (agree = AGREE_MAXIMISE):
  # exercises nondominated.h case AGREE_MAXIMISE (lines 634, 637)
  pts_max4 <- matrix(c(1,1,1,1, 0,0,0,0, 2,0,2,0), nrow = 3L, ncol = 4L, byrow = TRUE)
  expect_true(any_dominated(pts_max4, maximise = TRUE))
  expect_false(any_dominated(filter_dominated(pts_max4, maximise = TRUE), maximise = TRUE))
})

test_that("is_nondominated 4D large Pareto front", {
  # Large 4D Pareto front with dominated copies exercises kung_merge_dim3
  # (nondominated_kung.h lines 282-283: dominated S points removed in merge).
  # The r_size * s_size product must exceed KUNG_MERGE_THRESHOLD (1024) so
  # that kung_merge_dim3 is called instead of the brute-force fallback.
  t_vals <- seq(0.01, 0.99, length.out = 20L)
  s_vals <- seq(0.01, 0.99, length.out = 20L)
  grid <- expand.grid(t = t_vals, s = s_vals)
  front_4d <- cbind(grid$t, 1 - grid$t, grid$s, 1 - grid$s)
  dominated_4d <- front_4d[1:20, ] + 0.001
  pts_4d <- rbind(front_4d, dominated_4d)
  n_front <- nrow(front_4d)
  result_4d <- is_nondominated(pts_4d)
  expect_true(all(result_4d[seq_len(n_front)]))
  expect_false(any(result_4d[n_front + seq_len(nrow(dominated_4d))]))
  expect_false(any_dominated(pts_4d[result_4d, , drop = FALSE]))
})

test_that("is_nondominated 5D same-dim kung_merge_nobase", {
  # 5D data with a constant obj2 value per half exercises kung_merge_nobase
  # (nondominated_kung.h line 602: r2_size == 0 && s2_size == 0 branch).
  #   R half: (t, 0.5, a, 1-a, 0)  t in [0.01,0.49], a in [0.01,0.99]
  #   S half: (t, 1.0, a, 1-a, 0)  t in [0.51,0.99], a in [0.02,0.98]
  # After Kung shifts to obj2, all S have obj2=1.0 (s2_size=0) and all R have
  # obj2=0.5 <= 1.0 (r2_size=0), triggering line 602.
  n5 <- 50L
  t_r <- seq(0.01, 0.49, length.out = n5)
  a_r <- seq(0.01, 0.99, length.out = n5)
  r_pts <- cbind(t_r, 0.5, a_r, 1 - a_r, 0)
  t_s <- seq(0.51, 0.99, length.out = n5)
  a_s <- seq(0.02, 0.98, length.out = n5)  # distinct from a_r
  s_pts <- cbind(t_s, 1.0, a_s, 1 - a_s, 0)
  pts_5d <- rbind(r_pts, s_pts)
  result_5d <- is_nondominated(pts_5d)
  expect_true(all(result_5d))
  expect_false(any_dominated(pts_5d))
})
