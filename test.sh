#!/bin/bash

BINARY=$1
SPACKFILE=$2
export PATH="./:$PATH"
export CURDIR=$(pwd)

export FAILURES=0

##
# Perform some pre-flight checks
SRC="$CURDIR/sunpack.c"
if [ ! -f $SRC ]; then
    echo "Failed to find source code!"
    echo $SRC
    exit 1
fi

if [ "$USER" == "" ]; then
    USER=$(dd if=/dev/urandom bs=8 count=2 2>/dev/null|md5sum|awk '{print $1}'|cut -b-8)
    export USER
fi
TMPWORKDIR=$(mktemp -d /tmp/$USER-spack-test-XXXXXX)
export TMPWORKDIR
if [ ! -d $TMPWORKDIR ]; then
    echo "Temp Dir not found!"
    exit 1
fi
echo $TMPWORKDIR|grep '^/tmp/' >/dev/null
STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "Something went wrong creating tmp dir!"
    exit 1
fi

export TMPREPORT="$TMPWORKDIR/report.txt"

function log {
    echo $1 >> $TMPREPORT
}

function runtest {
    TEST=$1
    if [ "$2" != "" ]; then
        MSG=$2
    else
        MSG=$1
    fi
    echo -n "[    ] $MSG"
    $TEST >/dev/null 2>&1
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        echo -e "\r[[31mFAIL[0m] $MSG - return status ${STATUS}"
        FAILURES=$(( $FAILURES + 1 ))
    else
        echo -e "\r[[32m OK [0m] $MSG"
    fi
}

#This test checks to make sure comment
#header was updated appropriately.
function headercheck {
    ERROR=0
    grep 'Date:' $SRC|grep -v 3015 
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=1
        log "Header check failed, you didn't update the Date!"
    fi
    grep Author $SRC |grep -v 'Doug Stan'|grep -v 'some.student'
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=2
        log "Header check failed, you didn't change the Author!"
    fi

    return $ERROR
}
#this function makes sure the program fails appropriately
function checkbadflags {
    ERROR=0
    
    #check no args
    $BINARY 
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=1
        log "$BINARY didn't fail with no command line arguments as expected!"
    fi

    $BINARY -l
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=2
        log "$BINARY didn't fail when spack file omitted!"
    fi

    $BINARY -l -x ./
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=3
        log "$BINARY didn't fail when wrong combination of arguments given!"
    fi

    $BINARY -x ./
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=4
        log "$BINARY didn't fail when spack file omitted!"
    fi

    $BINARY -x ./ -f
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=5
        log "$BINARY didn't fail when spack file omitted!"
    fi

    return ${ERROR}
}

function checkerrorhandling {
    ERROR=0

    #give it an invalid spack file
    $BINARY -l /dev/null
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=1
        log "Code is not checking for a valid spack file!!"
        log "  Add code to check if the file specified is an actual valid spack file. If not, your code should fail appropriately!"
    fi

    return ${ERROR}
}

function checkcorrectoutput {
    ERROR=0

    #give it an invalid spack file
    $BINARY -l ${SPACKFILE}
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=1
        log "Command failed when it shouldn't have"
    fi

cat <<EOF | diff - <($BINARY -l $SPACKFILE|grep -v '^Listing files in')
69632               	wallet.dat
24083               	doge2.jpg
61455               	test.html
348                 	md5sums
704                 	secret.txt
77                  	test.c
6662                	a.out
792029              	sample.gif
113911              	image.jpg
EOF
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=2
        log "Your code didn't list the files correclty."
        log "  are you sure you're using the print_record function?"
    fi

    return ${ERROR}

}

function checkforfakeoutput {
    ERROR=0

    grep 'md5sums' $SRC
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=$(( $ERROR + 1 ))
    fi

    grep 'doge2.jpg' $SRC
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=$(( $ERROR + 1 ))
    fi

    grep 'test.html' $SRC
    STATUS=$?
    if [ ${STATUS} -eq 0 ]; then
        ERROR=$(( $ERROR + 1 ))
    fi

    if [ ${ERROR} -gt 0 ]; then
        log "It seems you're trying to fake correct output!"
    fi
    return ${ERROR}
}

function checkextract {
    ERROR=0

    ALLDIR=$(mktemp -d "$TMPWORKDIR/all.XXXX")
    FILESDIR=$(mktemp -d "$TMPWORKDIR/files.XXXX")
    
    $BINARY -x $ALLDIR $SPACKFILE
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=1
        log "Extract all failed!"
    fi

    pushd $ALLDIR
    md5sum -c md5sums 
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=$(( $ERROR + 2 ))
        log "MD5sum check failed"
    fi
    popd

    $BINARY -x $FILESDIR -f md5sums $SPACKFILE
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=$(( $ERROR + 4 ))
        log "Extract single file failed!"
        log "  failed to extract md5sums file!"
    fi
    md5sum $FILESDIR/md5sums |grep 'df9bca427ba49a35dee4330f702e99ef'
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=$(( $ERROR + 8 ))
        log "Extract single file failed!"
        log "  md5sums file is corrupt! It's md5 checksum doesn't match!"
    fi

    FILES="secret.txt doge2.jpg image.jpg a.out wallet.dat sample.gif test.c test.html"
    for f in $FILES;do
        $BINARY -x $FILESDIR -f $f $SPACKFILE
        STATUS=$?
        if [ ${STATUS} -ne 0 ]; then
            ERROR=$(( $ERROR + 16 ))
            log "Extract single file failed!"
            log "  failed to extract $f file!"
        fi
    done

    pushd $FILESDIR
    md5sum -c md5sums 
    STATUS=$?
    if [ ${STATUS} -ne 0 ]; then
        ERROR=$(( $ERROR + 32))
        log "MD5sum check failed"
    fi
    popd
    
    return ${ERROR}
}


log "Sunpack Tests Starting $(date)"
log "###################################################"

##
# Run all tests
if [ -f $SRC ]; then
    runtest headercheck "Checking Comment Header"
    runtest checkforfakeoutput "Checking for faked output"
else
    echo "Can't find $SRC to check it!"
    log "$SRC missing!"
    exit 1
fi

if [ -x $BINARY ]; then
    runtest checkbadflags "Checking if wrong args causes failure."
    runtest checkerrorhandling "Checking if appropriate error handling is used."
    runtest checkcorrectoutput "Checking for correct program output."
    runtest checkextract "Checking that extract works."
else
    echo "$BINARY not executable or not found!"
    log "$BINARY missing!"
    exit 1
fi

#Clean up
log "###################################################"
log ""
log "Spack Tests Finished..."
log ""
if [ $FAILURES -gt 0 ]; then
    echo -e "\nErrors reported, following is the error report:\n"
    cat $TMPREPORT
    #mv $TMPREPORT $CURDIR
else
    echo "No errors reported, you're good to go!"
fi

echo "All done, removing temporary files in $TMPWORKDIR"
rm -rf $TMPWORKDIR
