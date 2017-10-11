function Controller() {
    installer.autoRejectMessageBoxes();
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.FinishButton);
    })

    var required_keys = ["dr_qt_path", "dr_qt_ver", "dr_packages"];
    for (var i = 0; i < required_keys.length; i++) {
        if (!installer.containsValue(required_keys[i])) {
            console.log("ERROR: " + required_keys[i] + " missing!")
            var warningLabel = gui.currentPageWidget().WarningLabel;
            warningLabel.setText("ERROR: " + required_keys[i] + " missing!");
            installer.autoAcceptMessageBoxes();
            gui.clickButton(buttons.CancelButton);
            return;
        }
    }
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
    var qt_path = installer.value('dr_qt_path');
    gui.currentPageWidget().TargetDirectoryLineEdit.setText(qt_path);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    var packages = installer.value("dr_packages").split(",");
    var ver = installer.value("dr_qt_ver");

    gui.currentPageWidget().deselectAll();
    packages.forEach(function(package) {
        if (package.startsWith('qt.')) // tools etc.
            gui.currentPageWidget().selectComponent(package);
        else // versioned Qt packages
            gui.currentPageWidget().selectComponent("qt." + ver + "." + package);
    })
    gui.currentPageWidget().selectComponent("qt.tools.qtcreator"); // qbs

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
