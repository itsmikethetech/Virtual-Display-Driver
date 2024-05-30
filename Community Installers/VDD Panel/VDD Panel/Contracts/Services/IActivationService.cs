namespace VDD_Panel.Contracts.Services;

public interface IActivationService
{
    Task ActivateAsync(object activationArgs);
}
