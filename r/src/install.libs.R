files <- c(
  "dominatedsets",
  "eaf",
  "epsilon",
  "hv",
  "hvapprox",
  "igd",
  "ndsort",
  "nondominated"
)

file_move <- function(from, to) {
  file.copy(from = from, to = to, overwrite = TRUE)
  file.remove(from)
}

if (WINDOWS) files <- paste0(files, ".exe")
if (any(file.exists(files))) {
  dest <- file.path(R_PACKAGE_DIR,  paste0('bin', R_ARCH))
  dir.create(dest, recursive = TRUE, showWarnings = FALSE)
  file_move(files, dest)
}

files <- Sys.glob(paste0("*", SHLIB_EXT))
if (any(file.exists(files))) {
  dest <- file.path(R_PACKAGE_DIR, paste0('libs', R_ARCH))
  dir.create(dest, recursive = TRUE, showWarnings = FALSE)
  file.copy(files, dest, overwrite = TRUE)
}
if (file.exists("symbols.rds"))
  file.copy("symbols.rds", dest, overwrite = TRUE)
