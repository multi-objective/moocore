% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/largest_eafdiff.R
\name{largest_eafdiff}
\alias{largest_eafdiff}
\title{Identify largest EAF differences}
\usage{
largest_eafdiff(x, maximise = FALSE, intervals = 5L, reference, ideal = NULL)
}
\arguments{
\item{x}{\code{list()}\cr A list of matrices or data frames with at least 3 columns (last column indicates the set).}

\item{maximise}{\code{logical()}\cr Whether the objectives must be maximised
instead of minimised. Either a single logical value that applies to all
objectives or a vector of logical values, with one value per objective.}

\item{intervals}{\code{integer(1)}\cr The absolute range of the differences
\eqn{[0, 1]} is partitioned into the number of intervals provided.}

\item{reference}{\code{numeric()}\cr Reference point as a vector of numerical
values.}

\item{ideal}{\code{numeric()}\cr Ideal point as a vector of numerical values.  If
\code{NULL}, it is calculated as minimum (or maximum if maximising that
objective) of each objective in the input data.}
}
\value{
\code{list()}\cr A list with two components \code{pair} and \code{value}.
}
\description{
Given a list of datasets, return the indexes of the pair with the largest
EAF differences according to the method proposed by \citet{DiaLop2020ejor}.
}
\examples{
# FIXME: This example is too large, we need a smaller one.
data(tpls50x20_1_MWT)
nadir <- apply(tpls50x20_1_MWT[,2:3], 2L, max)
x <- largest_eafdiff(split.data.frame(tpls50x20_1_MWT[,2:4], tpls50x20_1_MWT[, 1L]),
                     reference = nadir)
str(x)

}
\references{
\insertAllCited{}
}
\concept{eaf}
