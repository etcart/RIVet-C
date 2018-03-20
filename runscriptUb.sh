clean(){
	while [ "$1" ]; do
		if [ -d "$1" ]; then
			clean "$1"/*
		else
			python shittyballs.py "$1"
			cd ../RIVet
			./RIVread ../bookCleaner/cleanbooks/
			cd ../bookCleaner
			rm  -r cleanbooks/
		fi
		shift
	done
}

clean books/*
