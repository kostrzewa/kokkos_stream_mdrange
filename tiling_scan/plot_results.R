require(hadron)
require(dplyr)
require(ggplot2)

archs <- c(#"dual_epyc3_7713_gcc",
           #"dual_epyc3_7713_clang",
           "dual_epyc2_7742_gcc",
           "dual_xeon_8468_gcc", 
           "dual_xeon_8468_clang", 
           "nvidia_a100", 
           "amd_mi250")

bws <- data.frame(bw=c(#204.8, 2*204.8,
                       #204.8, 2*204.8,
                       2*204.8,
                       2*307.2,
                       2*307.2,
                       1555,
                       1638),
                  ylim=c(#600, 600,
                         #600, 600,
                         600, #600,
                         900, #900,
                         900,
                         1700,
                         1750),
                  architecture = c(#"dual_epyc3_7713_gcc", "dual_epyc3_7713_gcc",
                                   #"dual_epyc3_7713_clang", "dual_epyc3_7713_clang",
                                   "dual_epyc2_7742_gcc", #"dual_epyc2_7742_gcc",
                                   "dual_xeon_8468_gcc", #"dual_xeon_8468_gcc",
                                   "dual_xeon_8468_clang", #"dual_xeon_8468_clang",
                                   "nvidia_a100", 
                                   "amd_mi250"),
                  nt=c(#64, 128,
                       #64, 128,
                       128,
                       96, 
                       96, 
                       32, 
                       7))

tikzfiles <- hadron::tikz.init("kokkos_stream_mdrange_tiling_scan", width=14, height=6)

for( i in 1:length(archs) ){
  arch <- archs[i]
  bwdat <- dplyr::filter(bws, architecture == arch)
  dat <- read.table(sprintf("%s/results.dat", arch), header=TRUE)
  ncol <- length(unique(dat$factor))
  maxdat <- dplyr::group_by(dat, factor, policy, nt) %>%
            dplyr::filter(n == max(n)) %>%
            dplyr::filter(bw == max(bw)) %>%
            dplyr::ungroup()

  p <- ggplot2::ggplot(dat, aes(x = n*8*10^(-6), y = bw, colour = kernel, shape = kernel)) +
       ggplot2::geom_line() +
       ggplot2::geom_point() +
       ggplot2::geom_hline(data = bwdat, aes(yintercept = bw), colour = "blue") +
       ggplot2::geom_hline(data = maxdat, aes(yintercept = bw), colour = "red") +
       ggplot2::geom_point(data = bwdat, shape = NA, fill = NA, colour = NA, x = 1,
                           aes(y = bw)) +
       ggplot2::geom_label(data = bwdat, 
                           aes(label = sprintf("mem %0.0f GB/s", bw),
                               y = bw, x = 0.03),
                           colour = "blue",
                           inherit.aes=FALSE, 
                           size = 2.5) +
       ggplot2::geom_label(data = maxdat, 
                           aes(label = sprintf("%0.0f GB/s", bw),
                               y = bw, x = 0.03),
                           colour = "red",
                           inherit.aes=FALSE, 
                           size = 2.5) +
       ggplot2::ggtitle(hadron::escapeLatexSpecials(sprintf("%s, OMP_PROC_BIND=close, OMP_PLACES=cores", arch))) +
       ggplot2::facet_wrap(sprintf("policy = %s", policy) ~ sprintf("tiling factor = %02d", factor), ncol = ncol) +
       ggplot2::scale_x_continuous(trans = "log10") +
       ggplot2::coord_cartesian(ylim = c(0, bwdat$ylim[1])) +
       ggplot2::labs(x = "vector size [MB]",
                     y = "BW [GB/s]") +
       ggplot2::theme_bw() +
       ggplot2::theme(plot.title = element_text(color="blue", size = 10, face="bold")) 
  plot(p)
}

hadron::tikz.finalize(tikzfiles, crop=FALSE)
