for filename in ./*.png; do
    toktx --verbose --t2 --encode uastc ${filename::-4} $filename
done
