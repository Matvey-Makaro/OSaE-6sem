argc=$#

true="1"
false="0"

is_type=$false
is_file=$false
is_id=$false

flag_type="-t"
flag_file="-f"
flag_id="-i"

is_type_defined=$false
is_file_defined=$false
is_id_defined=$false


UNKNOWN_ARG_ERROR="1"
INCORRECT_NUMBER_OF_ARGUMENTS="2"

api_dev_key_value="fG2Ysh3Xk73DXjnVth0xqPNE_HK2k42Z"
api_user_name_value="matvey-makaro"
api_user_password_value="ma1ka2ro3ma1ka2ro3"


function print_error() {
    echo "Unknown argument: $arg"
    exit $UNKNOWN_ARG_ERROR
}


if [ $(($argc % 2)) -eq 1 ]; then
    echo "Incorrect number of arguments. Argc must be even."
    exit $INCORRECT_NUMBER_OF_ARGUMENTS
fi

for arg in $*
do
    if [ $is_type -eq $true ]; then
        type=$arg
        is_type=$false
        is_type_defined=$true
    elif [ $is_file -eq $true ]; then
        file=$arg
        is_file=$false
        is_file_defined=$true
    elif [ $is_id -eq $true ]; then
        id=$arg
        is_id=$false
        is_id_defined=$true
    elif [ "$arg" = "$flag_type" ]; then
        is_type=$true
    elif [ "$arg" = "$flag_file" ]; then
        is_file=$true
    elif [ "$arg" = "$flag_id" ]; then
        is_id=$true
    else
        print_error
    fi
done

api_user_key_value=`curl -sS -X POST -d "api_dev_key=${api_dev_key_value}" -d "api_user_name=${api_user_name_value}" -d "api_user_password=${api_user_password_value}" "https://pastebin.com/api/api_login.php"`

if [ $is_id_defined -eq $true ]; then
    text=`curl -sS -X POST -d "api_dev_key=${api_dev_key_value}" -d "api_user_key=${api_user_key_value}" -d "api_option=show_paste" -d "api_paste_key=${id}" "https://pastebin.com/api/api_post.php"`
    
    if [ $is_file_defined -eq $true ]; then
        exec 1>${file}
    fi
    echo "$text"

else
    if [ $is_file_defined -eq $true ]; then
        text=$(<${file})
    else
        read -p "Enter text: " text
    fi

    if [ $is_type_defined -ne $true ]; then
        if [ $is_file_defined -eq $true ]; then
            type="${file##*.}"
        else
            type=""
        fi
    fi
    
    res=`curl -sS -X POST -d "api_dev_key=${api_dev_key_value}" -d "api_user_key=${api_user_key_value}" -d "api_paste_code=${text}" -d "api_paste_format=${type}" -d "api_option=paste" "https://pastebin.com/api/api_post.php"`
    echo "$res"
fi



