using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading; // 添加 Dispatcher 命名空间

namespace TimeLineControlPanel
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            // 为所有时间输入框添加粘贴事件处理
            DataObject.AddPastingHandler(TaskTimeTextBoxHour1, TextBox_Pasting);
            DataObject.AddPastingHandler(TaskTimeTextBoxHour2, TextBox_Pasting);
            DataObject.AddPastingHandler(TaskTimeTextBoxMin1, TextBox_Pasting);
            DataObject.AddPastingHandler(TaskTimeTextBoxMin2, TextBox_Pasting);
        }

        private void TextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            TextBox textBox = sender as TextBox;
            if (!int.TryParse(e.Text, out int value) || value < 0 || value > 9)
            {
                e.Handled = true;
                return;
            }
        }

        private void TextBox_Pasting(object sender, DataObjectPastingEventArgs e)
        {
            // 处理粘贴操作，只允许单个数字
            if (e.DataObject.GetDataPresent(typeof(string)))
            {
                string text = (string)e.DataObject.GetData(typeof(string));
                if (!int.TryParse(text, out int value) || value < 0 || value > 9 || text.Length > 1)
                {
                    e.CancelCommand(); // 阻止粘贴
                }
            }
            else
            {
                e.CancelCommand(); // 非字符串数据，阻止粘贴
            }
        }

        private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            TextBox textBox = sender as TextBox;
            if (textBox == null) return;

            // 检查输入内容是否为单个数字
            if (!int.TryParse(textBox.Text, out int value) || value < 0 || value > 9)
            {
                // 如果输入非法（包括中文、英文或其他字符），恢复为 "0"
                textBox.Text = "0";
                textBox.SelectAll(); // 选中内容，便于用户继续输入
                return;
            }

            // 处理 TaskTimeTextBoxMin1 大于 5 的情况
            if (textBox == TaskTimeTextBoxMin1 && value > 5)
            {
                textBox.Text = (value - 6).ToString(); // TaskTimeTextBoxMin1 减 6
                int hour2 = int.TryParse(TaskTimeTextBoxHour2.Text, out int h2) ? h2 : 0;
                hour2 += 1; // TaskTimeTextBoxHour2 加 1

                // 处理小时进位
                if (hour2 >= 10)
                {
                    int hour1 = int.TryParse(TaskTimeTextBoxHour1.Text, out int h1) ? h1 : 0;
                    hour1 += hour2 / 10; // 向 Hour1 进位
                    if (hour1 >= 10)
                    {
                        TaskTimeTextBoxHour1.Text = "9";
                        TaskTimeTextBoxHour2.Text = "9";
                        TaskTimeTextBoxMin1.Text = "5";
                        TaskTimeTextBoxMin2.Text = "9";
                        MessageBox.Show("时间已达最大值 99:59，无法继续增加！");
                        textBox.SelectAll(); // 重新选中
                        return;
                    }
                    
                    hour2 %= 10; // 保留 Hour2 的个位
                    TaskTimeTextBoxHour1.Text = hour1.ToString();
                    TaskTimeTextBoxHour2.Text = hour2.ToString();
                    
                }
                else
                {
                    TaskTimeTextBoxHour2.Text = hour2.ToString();
                }

                textBox.SelectAll(); // 重新选中
            }
        }

        private void TextBox_GotFocus(object sender, RoutedEventArgs e)
        {
            TextBox textBox = sender as TextBox;
            if (textBox != null)
            {
                Dispatcher.BeginInvoke(DispatcherPriority.Input, new Action(() =>
                {
                    textBox.SelectAll();
                }));
            }
        }

        private void TextBox_PreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            TextBox textBox = sender as TextBox;
            if (textBox != null)
            {
                if (!textBox.IsKeyboardFocusWithin)
                {
                    textBox.Focus();
                }
                e.Handled = true; // 阻止默认鼠标行为
                Dispatcher.BeginInvoke(DispatcherPriority.Input, new Action(() =>
                {
                    textBox.SelectAll(); // 每次单击都重新选中
                }));
            }
        }

        private void StartButton_Click(object sender, RoutedEventArgs e)
        {
            // 获取输入
            string taskName = TaskNameTextBox.Text.Trim();

            // 获取时间
            int hour1 = int.TryParse(TaskTimeTextBoxHour1.Text, out int h1) ? h1 : 0;
            int hour2 = int.TryParse(TaskTimeTextBoxHour2.Text, out int h2) ? h2 : 0;
            int min1 = int.TryParse(TaskTimeTextBoxMin1.Text, out int m1) ? m1 : 0;
            int min2 = int.TryParse(TaskTimeTextBoxMin2.Text, out int m2) ? m2 : 0;

            

            // 验证小时和分钟范围
            int hours = hour1 * 10 + hour2;
            int minutes = min1 * 10 + min2;

            int taskTimeText = hour1 * 36000 + hour2 * 3600 + min1 * 600 + min2 ;
            int mode = ModeComboBox.SelectedIndex; // 0: 渐短, 1: 渐长

            // 验证输入
            if (string.IsNullOrEmpty(taskName))
            {
                MessageBox.Show("请输入任务名称！", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            if (taskTimeText <= 0)
            {
                MessageBox.Show("请输入有效的任务时间！", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            try
            {
                // 构建 TimeLineWidget.exe 路径
                string widgetPath = System.IO.Path.Combine("D:\\Project\\ToDoList\\TimeLine2\\TimeLine2\\x64\\Debug", "TimeLineWidget.exe");

                // 启动 TimeLineWidget，传递参数
                ProcessStartInfo startInfo = new ProcessStartInfo
                {
                    FileName = widgetPath,
                    Arguments = $"\"{taskName}\" {taskTimeText} {mode}",
                    UseShellExecute = false,
                    CreateNoWindow = true
                };
                Process.Start(startInfo);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"启动小组件失败：{ex.Message}", "错误", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}