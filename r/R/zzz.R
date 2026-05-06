.libmoocore_constants <- NULL

.onLoad <- function(lib, pkg) {
  .libmoocore_constants <<-  .Call("libmoocore_constants")
  Rdpack::Rdpack_bibstyles(package = pkg, authors = "LongNames")
  invisible(NULL)
}
