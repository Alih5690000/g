for f in $(ls frame*.png | sort -Vr); do
    num=${f#frame}
    num=${num%.png}
    new=$((num + 1))
    mv "$f" "frame$new.png"
done
