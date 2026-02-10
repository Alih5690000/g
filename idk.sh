mkdir shifted
for img in *.png; do
    convert "$img" -background none -gravity northwest -geometry +10+20 miff:- shifted/"$img"
done

convert llegs/*.png -transparent "#FFFFFF" plr_animLegsWalking/frame%d.png

i=0
for f in *.png; do
  mv "$f" "frame$i.png"
  ((i++))
done

