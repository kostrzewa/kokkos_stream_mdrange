export MODULEPATH=/opt/software/easybuild-AMD/modules/all:/etc/modulefiles:/usr/share/modulefiles:/opt/software/modulefiles:/usr/share/modulefiles/Linux:/usr/share/modulefiles/Core:/usr/share/lmod/lmod/modulefiles/Core
module purge
module load \
  GCCcore/12.3.0 \
  CUDA/12.1.1 \
  CMake/3.26.3-GCCcore-12.3.0

