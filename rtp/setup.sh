# ffmpeg 
sudo apt update -qq && sudo apt -y install \
  autoconf \
  automake \
  build-essential \
  cmake \
  git-core \
  libass-dev \
  libfreetype6-dev \
  libgnutls28-dev \
  libmp3lame-dev \
  libsdl2-dev \
  libtool \
  libva-dev \
  libvdpau-dev \
  libvorbis-dev \
  libxcb1-dev \
  libxcb-shm0-dev \
  libxcb-xfixes0-dev \
  meson \
  ninja-build \
  pkg-config \
  texinfo \
  wget \
  yasm \
  zlib1g-dev


sudo apt install libx265-dev libnuma-dev libx264-dev pkg-config libavcodec-dev ffmpeg

#git clone https://github.com/FFmpeg/FFmpeg.git 
#cd FFmpeg
#./configure --enable-shared --enable-libx265 --enable-encoder=libx265 --enable-gpl --enable-libx264 --enable-encoder=libx264 --arch=aarch64
#make -j
