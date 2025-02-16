require(hadron)
require(dplyr)
require(ggplot2)

archs <- c("dual_epyc3_7713", "dual_xeon_8468", "a100")
bws <- data.frame(bw=c(190.73, 2*190.73,
                       307.2, 2*307.2,
                       1935),
                  architecture = c("dual_epyc3_7713", "dual_epyc3_7713",
                                   "dual_xeon_8468", "dual_xeon_8468",
                                   "a100"),
                  nt=c(64, 128, 48, 96, 32))

tikzfiles <- hadron::tikz.init("kokkos_stream_mdrange", width=6, height=4)

for( i in 1:length(archs) ){
  arch <- archs[i]
  bwdat <- dplyr::filter(bws, architecture == arch) 
  dat <- read.table(sprintf("%s/results.dat", arch), header=TRUE)
  p <- ggplot2::ggplot(dat, aes(x = n^4*8*10^(-6), y = bw, colour = kernel, shape = kernel)) +
       ggplot2::geom_line() +
       ggplot2::geom_point() +
       ggplot2::geom_hline(data = bwdat, aes(yintercept = bw), colour = "blue") +
       ggplot2::geom_point(data = bwdat, shape = NA, fill = NA, colour = NA, x = 1,
                           aes(y = bw)) +
       ggplot2::geom_label(data = bwdat, 
                           aes(label = sprintf("mem %0.0f GB/s", bw),
                               y = bw+100, x = 0.03),
                           colour = "blue",
                           inherit.aes=FALSE, 
                           size = 2.5) +
       ggplot2::ggtitle(hadron::escapeLatexSpecials(sprintf("%s, OMP_PROC_BIND=close, OMP_PLACES=cores", arch))) +
       ggplot2::facet_wrap(sprintf("$n_{\\textrm{th}}=%03d$",nt) ~ sprintf("policy = %s", policy)) +
       #ggplot2::scale_y_continuous(trans = "log10") +
       ggplot2::scale_x_continuous(trans = "log10") +
       ggplot2::labs(x = "vector size [MB]",
                     y = "BW [GB/s]") +
       ggplot2::theme_bw() +
       ggplot2::theme(plot.title = element_text(color="blue", size = 10, face="bold")) 
  plot(p)
}

hadron::tikz.finalize(tikzfiles, crop=FALSE)
