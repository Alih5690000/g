read -p "Enter file name " filename
read -p "Enter output path " output_path
ffmpeg -i "$filename" "$output_path"/frame%d.png