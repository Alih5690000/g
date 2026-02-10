read -p "Enter folder name " filename
read -p "Enter output path " output_path
convert "$filename/*.png" -transparent "#FFFFFF" "$output_path"/frame%d.png