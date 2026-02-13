read -p "Enter file name " filename
read -p "Enter output path " output_path
ffmpeg -i "$filename" -start_number 1 "$output_path"/frame%d.png