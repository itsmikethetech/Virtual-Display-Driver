using Microsoft.UI.Xaml.Controls;

using VDD_Panel.ViewModels;

namespace VDD_Panel.Views;

public sealed partial class ConfigurePage : Page
{
    public ConfigureViewModel ViewModel
    {
        get;
    }

    public ConfigurePage()
    {
        ViewModel = App.GetService<ConfigureViewModel>();
        InitializeComponent();
    }
}
