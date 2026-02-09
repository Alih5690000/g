mkdir shifted
for img in *.png; do
    convert "$img" -background none -gravity northwest -geometry +10+20 miff:- shifted/"$img"
done
