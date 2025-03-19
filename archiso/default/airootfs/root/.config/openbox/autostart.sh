#sh ~/.local/bin/setbg
#exec ~/ali

# Programs that will run after Openbox has started

# Set the wallpaper
hsetroot ~/wallpapers/bg.jpg &

# Run a Composite manager
xcompmgr -c -t-5 -l-5 -r4.2 -o.55 &

# SCIM support (for typing non-english characters)
scim -d &

# A panel for good times
fbpanel &

exec ~/ali &