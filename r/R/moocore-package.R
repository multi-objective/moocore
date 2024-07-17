#' @keywords internal
"_PACKAGE"

## usethis namespace: start
#' @importFrom matrixStats colRanges
#' @importFrom Rdpack reprompt
#' @importFrom utils modifyList write.table tail
#' @useDynLib moocore, .registration = TRUE
## usethis namespace: end
NULL

#' Results of Hybrid GA on Vanzyl and Richmond water networks
#'
#'@format
#'  A list with two data frames, each of them with three columns, as
#'  produced by [read_datasets()].
#'  \describe{
#'    \item{`$vanzyl`}{data frame of results on Vanzyl network}
#'    \item{`$richmond`}{data frame of results on Richmond
#'      network. The second column is filled with `NA`}
#'  }
#'
#'@source \insertRef{LopezIbanezPhD}{moocore}.
#'
#' @examples
#'data(HybridGA)
#'print(HybridGA$vanzyl)
#'print(HybridGA$richmond)
#' @keywords datasets
"HybridGA"

#'Results of SPEA2 when minimising electrical cost and maximising the
#'minimum idle time of pumps on Richmond water network.
#'
#'@format
#'  A data frame as produced by [read_datasets()]. The second
#'  column measures time in seconds and corresponds to a maximisation problem.
#'
#' @source \insertRef{LopezIbanezPhD}{moocore}
#'
#'@examples
#' data(HybridGA)
#' data(SPEA2minstoptimeRichmond)
#' SPEA2minstoptimeRichmond[,2] <- SPEA2minstoptimeRichmond[,2] / 60
#' # eafplot (SPEA2minstoptimeRichmond, xlab = expression(C[E]),
#' #          ylab = "Minimum idle time (minutes)", maximise = c(FALSE, TRUE),
#' #          las = 1, log = "y", legend.pos = "bottomright")
#' @keywords datasets
"SPEA2minstoptimeRichmond"

#' Results of SPEA2 with relative time-controlled triggers on Richmond water
#' network.
#'
#'@format
#'  A data frame as produced by [read_datasets()].
#'
#' @source \insertRef{LopezIbanezPhD}{moocore}
#'
#'@examples
#'data(HybridGA)
#'data(SPEA2relativeRichmond)
#' @keywords datasets
"SPEA2relativeRichmond"

#'Results of SPEA2 with relative time-controlled triggers on Vanzyl's
#'water network.
#'
#' @format
#'  A data frame as produced by [read_datasets()].
#'
#'@source \insertRef{LopezIbanezPhD}{moocore}
#'
#'@examples
#'data(HybridGA)
#'data(SPEA2relativeVanzyl)
#' @keywords datasets
"SPEA2relativeVanzyl"


#' Conditional Pareto fronts obtained from Gaussian processes simulations.
#'
#' The data has the only goal of providing an example of use of [vorobT()] and
#' [vorobDev()]. It has been obtained by fitting two Gaussian processes on 20
#' observations of a bi-objective problem, before generating conditional
#' simulation of both GPs at different locations and extracting non-dominated
#' values of coupled simulations.
#'
#' @format
#'  A data frame with 2967 observations on the following 3 variables.
#'  \describe{
#'    \item{`f1`}{first objective values.}
#'    \item{`f2`}{second objective values.}
#'    \item{`set`}{indices of corresponding conditional Pareto fronts.}
#'  }
#'
#'@source
#'
#' \insertRef{BinGinRou2015gaupar}{moocore}
#'
#'@examples
#' data(CPFs)
#' vorobT(CPFs, reference = c(2, 200))
#'@keywords datasets
"CPFs"

#' Various strategies of Two-Phase Local Search applied to the Permutation Flowshop Problem with Makespan and Weighted Tardiness objectives.
#'
#' @format
#'  A data frame with 1511 observations of  4 variables:
#'  \describe{
#'    \item{`algorithm`}{TPLS search strategy}
#'    \item{`Makespan`}{first objective values.}
#'    \item{`WeightedTardiness`}{second objective values.}
#'    \item{`set`}{indices of corresponding conditional Pareto fronts.}
#'  }
#'
#'@source
#'
#' \insertRef{DubLopStu2011amai}{moocore}
#'
#'@examples
#' data(tpls50x20_1_MWT)
#' str(tpls50x20_1_MWT)
#'@keywords datasets
"tpls50x20_1_MWT"
