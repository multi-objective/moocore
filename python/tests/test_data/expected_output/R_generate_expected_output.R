# Run from this folder
# Rscript R_generate_expected_output.R
library(eaf)

dat1 <- read_datasets("../input1.dat")
dat_sphere <- read_datasets("../spherical-250-10-3d.txt")
dat_uniform <- read_datasets("../uniform-250-10-3d.txt")
wrots_l10 <- read_datasets("../wrots_l10w100_dat.xz")
wrots_l100 <- read_datasets("../wrots_l100w10_dat.xz")
ALG_1_dat <- read_datasets("../ALG_1_dat.xz")

# These datasets are already in the form "Data + set number"
diff_points_100_1 <- read.table("../100_diff_points_1.txt")
diff_points_100_2 <- read.table("../100_diff_points_2.txt")

# get_eaf test fetch results
eaf_dat1 <- eafs(dat1[,1:2], dat1[,3])
eaf_dat_sphere <-  eafs(dat_sphere[,1:3], dat_sphere[,4])
eaf_dat_uniform <- eafs(dat_uniform[,1:3], dat_uniform[,4])
eaf_wrots_l10 <- eafs(wrots_l10[,1:2], wrots_l10[,3])
eaf_wrots_l100 <- eafs(wrots_l100[,1:2], wrots_l100[,3])
eaf_alg_1_dat <- eafs(ALG_1_dat[,1:2], ALG_1_dat[,3])

# get_eaf test with percentiles fetch results
eaf_dat1_pct <- eafs(dat1[,1:2], dat1[,3], percentiles=c(0,50,100))
eaf_dat_sphere_pct <-  eafs(dat_sphere[,1:3], dat_sphere[,4],percentiles=c(0,50,100))
eaf_dat_uniform_pct <- eafs(dat_uniform[,1:3], dat_uniform[,4], percentiles=c(0,50,100))
eaf_wrots_l10_pct <- eafs(wrots_l10[,1:2], wrots_l10[,3],percentiles=c(0,50,100))
eaf_wrots_l100_pct <- eafs(wrots_l100[,1:2], wrots_l100[,3], percentiles=c(0,50,100))
eaf_alg_1_dat_pct <- eafs(eaf_alg_1_dat[,1:2], eaf_alg_1_dat[,3], percentiles=c(0,50,100))

# get_diff_eaf tests
eaf_diff_point12 <- eafdiff(diff_points_100_1, diff_points_100_2)

# get_diff_eaf tests with intervals
eaf_diff_point12_int3 <- eafdiff(diff_points_100_1, diff_points_100_2, intervals=3)

# Write results
write.table(dat1, "read_datasets/dat1_read_datasets.txt", row.names = FALSE, col.names = FALSE)
write.table(dat_sphere, "read_datasets/spherical_read_datasets.txt", row.names = FALSE, col.names = FALSE)
write.table(dat_uniform, "read_datasets/uniform_read_datasets.txt", row.names = FALSE, col.names = FALSE)
write.table(wrots_l10, "read_datasets/wrots_l10_read_datasets.txt", row.names = FALSE, col.names = FALSE)
write.table(wrots_l100, "read_datasets/wrots_l100_read_datasets.txt", row.names = FALSE, col.names = FALSE)
write.table(ALG_1_dat, "read_datasets/ALG_1_dat_read_datasets.txt", row.names = FALSE, col.names = FALSE)
# corresponds to get_eaf function
write.table(eaf_dat1, "get_eaf/dat1_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_dat_sphere, "get_eaf/spherical_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_dat_uniform, "get_eaf/uniform_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_wrots_l10, "get_eaf/wrots_l10_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_wrots_l100, "get_eaf/wrots_l100_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_alg_1_dat, "get_eaf/ALG_1_dat_get_eaf.txt", row.names = FALSE, col.names = FALSE)
# EAF with percentile values (0,50,100)
write.table(eaf_dat1_pct, "get_eaf/pct_dat1_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_dat_sphere_pct, "get_eaf/pct_spherical_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_dat_uniform_pct, "get_eaf/pct_uniform_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_wrots_l10_pct, "get_eaf/pct_wrots_l10_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_wrots_l100_pct, "get_eaf/pct_wrots_l100_get_eaf.txt", row.names = FALSE, col.names = FALSE)
write.table(eaf_alg_1_dat_pct, "get_eaf/pct_ALG_1_dat_get_eaf.txt", row.names = FALSE, col.names = FALSE)

# get_diff_eaf tests
write.table(eaf_diff_point12, "get_diff_eaf/points12_get_diff_eaf.txt", row.names = FALSE, col.names = FALSE)

# get_diff_eaf tests with intervals
write.table(eaf_diff_point12_int3, "get_diff_eaf/int3_points12_get_diff_eaf.txt", row.names = FALSE, col.names = FALSE)
