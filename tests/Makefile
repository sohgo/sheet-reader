
test:
	./test-sheetreader.sh ./samples/fax-01.tif 2> /dev/null
	./test-sheetreader.sh ./samples/fax-02.tif 2> /dev/null
	./test-sheetreader.sh ./samples/fax-03.tif 2> /dev/null
	./test-sheetreader.sh ./samples/fax-04.tif 2> /dev/null
	./test-sheetreader.sh ./samples/fax-05.tif 2> /dev/null
	./test-sheetreader.sh ./samples/fax-06.tif 2> /dev/null

test2:
	../src/sheetreader -m sql -c ./receive_sheetreader -r UNNUMBER -s UNNUMBER -p ./analyzedimage/ -l ./etc/ samples/fax-ffhs-20170123155019.png | ./checkAnswer --correct-data correctData.csv --fax-image fax-ffhs-20170123155019.png | sort -k 5
	../src/sheetreader -m sql -c ./receive_sheetreader -r UNNUMBER -s UNNUMBER -p ./analyzedimage/ -l ./etc/ samples/fax-ffhs-20170123155035.png | ./checkAnswer --correct-data correctData.csv --fax-image fax-ffhs-20170123155035.png | sort -k 5
