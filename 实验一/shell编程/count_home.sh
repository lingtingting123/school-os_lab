#!/bin/bash

home_dir="/home"

output_file="file.ini"

# 清空或创建输出文件
> "${output_file}"

echo "[文件夹]" >> "${output_file}"

# ls -1以单列形式列出目录内容, 注意是数字1不是字母l
sudo ls -1 "${home_dir}" >> "${output_file}"

# 统计子目录数量
dir_count=$(sudo find "${home_dir}" -mindepth 1 -maxdepth 1 -type d | wc -l)
# -mindepth 1：不包含/home本身, -maxdepth 1：只查找/home直接子目录, -type d：只查找目录
# | wc -l：通过管道统计行数（即目录数量, $(...)：命令替换，将命令输出赋值给变量dir_count

#统计子文件数量
file_count=$(sudo find "${home_dir}" -mindepth 1 -maxdepth 1 -type f | wc -l)

echo "" >> "${output_file}"
echo "[Directories Count]" >> "${output_file}"
echo ${dir_count} >> "${output_file}"
echo "" >> "${output_file}"
echo "[File Count]" >> "${output_file}"
echo ${file_count} >> "${output_file}" 

echo "统计结果已保存到 $output_file"