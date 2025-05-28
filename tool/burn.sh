#!/bin/sh
basepath=$(cd `dirname ./`; pwd)
project_base=${basepath}

JLinkExe -device N32G435RB -if SWD -speed 4000 -autoconnect 1 -CommanderScript ${project_base}/jlink/app-flash.jlink
#JLinkExe -device N32G435RB -if SWD -speed 4000 -autoconnect 1 -CommanderScript ${project_base}/jlink/boot-flash.jlink