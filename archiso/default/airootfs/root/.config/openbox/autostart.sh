# Programs that will run after Openbox has started

# Run a Composite manager
xcompmgr -c -t-5 -l-5 -r4.2 -o.55 &

# Set the wallpaper
hsetroot -fill ~/wallpapers/bg.jpg &

cd ~/ali && exec ./ali &