echo "Enter the name of the file to prepare:"
read inname
echo "Enter the output folder name (non existing):"
read outfolder
mkdir $outfolder
mkdir __tmp__
source grind.sh $inname __tmp__
source rmbg.sh __tmp__ $outfolder
rm -rf __tmp__