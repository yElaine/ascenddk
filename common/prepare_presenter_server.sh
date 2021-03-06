#
#   2 Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#   3 Neither the names of the copyright holders nor the names of the
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#   =======================================================================
script_path="$( cd "$(dirname "$0")" ; pwd -P )"

app_name=$1
remote_host=$2
download_mode=$3

. ${script_path}/utils/scripts/func_util.sh

presenter_atlasdk_ip=""
presenter_view_ip=""


function parse_presenter_altasdk_ip()
{
    OLD_IFS_PRESENTER="${IFS}"
    IFS=$'\n'
    
    valid_ips=""
    for ip_info in `/sbin/ifconfig | grep "inet addr"`
    do
        IFS=${OLD_IFS_PRESENTER}
        for each_ip in ${ip_info}
        do
            key=`echo ${each_ip} | awk -F ':' '{print $1}'`
            value=`echo ${each_ip} | awk -F ':' '{print $2}'`
            if [[ ${key} == "addr" ]];then
                ip=${value}
                valid_ips="${valid_ips}\t${value}\n"
            elif [[ ${key} == "Mask" ]];then
                mask=${value}
            fi
        done
        if [[ ${ip}"X" == "X" ]];then
            IFS=$'\n'
            continue
        fi
        check_ips_in_same_segment ${ip} ${mask} ${remote_host}
        if [[ $? -eq 0 ]];then
            presenter_atlasdk_ip=${ip}
            echo "Find ${presenter_atlasdk_ip} which is in the same segment with ${remote_host}."
            break
        fi
        IFS=$'\n'
    done
    IFS="${OLD_IFS_PRESENTER}"

    
    if [[ ${presenter_atlasdk_ip}"X" != "X" ]];then
        return 0
    fi
    
    echo "Can not find ip in the same segment with ${remote_host}."
    while [[ ${presenter_atlasdk_ip}"X" == "X" ]]
    do
        echo -en "Current environment valid ip list:\n${valid_ips}Please choose one which can connect to Atlas DK Developerment Board:"
        read presenter_atlasdk_ip
        if [[ ${presenter_atlasdk_ip}"X" != "X" ]];then
            check_ip_addr ${presenter_atlasdk_ip}
            if [[ $? -ne 0 ]];then
                echo "Invlid ip, please choose again..."
                presenter_atlasdk_ip=""
            else
                ret=`/sbin/ifconfig | grep ${presenter_atlasdk_ip}`
                if [[ $? -ne 0 ]];then
                    presenter_atlasdk_ip=""
                fi
            fi
        fi
    done
    return 0
}

function parse_presenter_view_ip()
{
    OLD_IFS_PRESENTER="${IFS}"
    IFS=$'\n'
    
    valid_view_ips=""
    for ip_info in `/sbin/ifconfig | grep "inet addr"`
    do
        IFS=${OLD_IFS_PRESENTER}
        for each_ip in ${ip_info}
        do
            key=`echo ${each_ip} | awk -F ':' '{print $1}'`
            value=`echo ${each_ip} | awk -F ':' '{print $2}'`
            if [[ ${key} == "addr" ]];then
                if [[ ${value} != ${presenter_atlasdk_ip} ]];then
                    valid_view_ips="${valid_view_ips}\t${value}\n"
                    break
                fi
            fi
        done
        IFS=$'\n'
    done
    IFS=${OLD_IFS_PRESENTER}
    
    while [[ ${presenter_view_ip}"X" == "X" ]]
    do
        echo -en "Current environment valid ip list:\n${valid_view_ips}Please choose one to show the presenter in browser(default: 127.0.0.1):"
        read presenter_view_ip
        
        if [[ ${presenter_view_ip}"X" != "X" ]];then
            check_ip_addr ${presenter_view_ip}
            if [[ $? -ne 0 ]];then
                echo "Invlid ip, please choose again..."
                presenter_view_ip=""
            else
                ret=`/sbin/ifconfig | grep ${presenter_view_ip}`
                if [[ $? -ne 0 ]];then
                    echo "Invlid ip, please choose again..."
                    presenter_view_ip=""
                fi
            fi
        else
            presenter_view_ip="127.0.0.1"
        fi
    done
    return 0
}

function main()
{
    stop_pid=`ps -ef | grep "presenter_server\.py" | grep "${app_name}" | awk -F ' ' '{print $2}'`
    if [[ ${stop_pid}"X" != "X" ]];then
        echo -e "\033[33mNow do presenter server configuration, kill existing presenter process: kill -9 ${stop_pid}.\033[0m"
        kill -9 ${stop_pid}
    fi

    if [[ ${download_mode} != "local" ]];then
        echo "Install python3 libs: pip3 install -r ${script_path}/presenter/server/requirements..."
        pip3 install -r ${script_path}/presenter/server/requirements
        if [ $? -ne 0 ];then
            echo "ERROR: install python3 libs failed, please check your env."
            return 1
        fi
    fi
    
    parse_presenter_altasdk_ip
    parse_presenter_view_ip
    
    echo "Use ${presenter_atlasdk_ip} to connect to Atlas DK Developerment Board..."
    sed -i "s/presenter_server_ip=[0-9.]*/presenter_server_ip=${presenter_atlasdk_ip}/g" ${script_path}/presenter/server/${app_name}/config/config.conf
    
    echo "Use ${presenter_view_ip} to show information in browser..."
    sed -i "s/web_server_ip=[0-9.]*/web_server_ip=${presenter_view_ip}/g" ${script_path}/presenter/server/${app_name}/config/config.conf
    echo "Finish to prepare ${app_name} presenter server ip configuration."
}

main
