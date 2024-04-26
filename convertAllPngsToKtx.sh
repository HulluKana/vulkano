for filename in ./*\.png; do
    echo toktx --t2 --genmipmap --encode uastc ${filename::-3}ktx $filename
    toktx --t2 --genmipmap --encode uastc ${filename::-3}ktx $filename
done
