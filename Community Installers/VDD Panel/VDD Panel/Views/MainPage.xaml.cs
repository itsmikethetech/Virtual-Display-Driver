using Microsoft.UI.Xaml.Controls;

using VDD_Panel.ViewModels;

namespace VDD_Panel.Views;

public sealed partial class MainPage : Page
{
    public MainViewModel ViewModel
    {
        get;
    }

    public MainPage()
    {
        ViewModel = App.GetService<MainViewModel>();
        InitializeComponent();
    }
}
