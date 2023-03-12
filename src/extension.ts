/* eslint-disable @typescript-eslint/naming-convention */
import * as vscode from 'vscode';
import * as path from 'path';
var exec = require('child_process').execFile;
// Called once when creating extension
export function activate(context: vscode.ExtensionContext) {

	console.log('Created IncludeLess extension.');
	// Called everytime extension is used
	let disposable = vscode.commands.registerCommand('includeless.includeless', () => {
		let source_file_path = path.join(__dirname, "../src/tests/simple/main.cpp");
		let exe_file_path = path.join(__dirname, "../src/backend/main.exe");
		exec(exe_file_path, [source_file_path], (error: any, stdout: any, stderr: any) => {
			if (error) {
				console.log(`${stderr}`);
				return;
			}
			console.log(`${stdout}`);
		});
		const collection = vscode.languages.createDiagnosticCollection('test');
		if (vscode.window.activeTextEditor) {
			updateDiagnostics(vscode.window.activeTextEditor.document, collection);
		}
		/*context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(editor => {
			if (editor) {
				updateDiagnostics(editor.document, collection);
			}
		}));*/
		vscode.window.showInformationMessage('Removed all unused includes!');
	});

	context.subscriptions.push(disposable);
}

function updateDiagnostics(document: vscode.TextDocument, collection: vscode.DiagnosticCollection): void {
	if (document) {
		collection.set(document.uri, [{
			code: '',
			message: 'You can remove this Include',
			range: new vscode.Range(new vscode.Position(3, 4), new vscode.Position(3, 10)),
			severity: vscode.DiagnosticSeverity.Warning,
		}]);
	} else {
		collection.clear();
	}
}

export function deactivate() { }
