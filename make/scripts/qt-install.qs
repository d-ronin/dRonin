function Controller() {
    installer.autoRejectMessageBoxes();
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.FinishButton);
    })
}

Controller.prototype.WelcomePageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    var qt_path = installer.environmentVariable('dronin_qt_path');
    if (!qt_path.length) {
        console.log("ERROR: dronin_qt_path environment variable missing!")
        var warningLabel = gui.currentPageWidget().WarningLabel;
        warningLabel.setText("ERROR: dronin_qt_path environment variable missing!");
        installer.autoAcceptMessageBoxes();
        gui.clickButton(buttons.CancelButton);

    } else {
        gui.currentPageWidget().TargetDirectoryLineEdit.setText(qt_path);
        gui.clickButton(buttons.NextButton);
    }
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    gui.currentPageWidget().deselectComponent("qt.tools.qtcreator")
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function() {
    var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm
    if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
        checkBoxForm.launchQtCreatorCheckBox.checked = false;
    }
    gui.clickButton(buttons.FinishButton);
}
