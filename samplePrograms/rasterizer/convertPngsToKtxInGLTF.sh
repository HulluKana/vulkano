echo "$(jq '.["images"][].mimeType |= .[:-3] + "ktx" | .["images"][].uri |= .[:-3] + "ktx"' $1)" > $1
