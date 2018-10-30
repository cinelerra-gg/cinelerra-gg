# Render output mux-ed into a single file
ffmpeg -f concat -safe 0 -i <(for f in "$CIN_RENDER"0*; do echo "file '$f'"; done) -c copy -y $CIN_RENDER 
