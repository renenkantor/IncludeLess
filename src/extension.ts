/* eslint-disable @typescript-eslint/naming-convention */
import * as vscode from 'vscode';
import * as path from 'path';
import { readFileSync } from 'fs';

var exec = require('child_process').execFile;

// Called once when creating extension
export function activate(context: vscode.ExtensionContext) {
	console.log('Created IncludeLess extension.');
	// Called everytime extension is used
	let disposable = vscode.commands.registerCommand('includeless.includeless', async () => {
		let source_file_path : string;
		if (vscode.window.activeTextEditor !== undefined) {
			source_file_path = vscode.window.activeTextEditor.document.fileName;
			var fileExt = path.basename(source_file_path).split('.').pop();
			if (fileExt !== 'c' && fileExt !== 'cpp' && fileExt !== 'h' && fileExt !== 'cxx') {
				vscode.window.showInformationMessage('Extension can only be used on source files and headers');
				return;
			}
			
		} else {
			vscode.window.showInformationMessage('Open a source file to use the extension');
			return;
		}
		let exe_file_path = path.join(__dirname, "../src/backend/main.exe");
		const execPromise = new Promise((resolve, reject) => {
			exec(exe_file_path, [source_file_path], (error: any, stdout: any, stderr: any) => {
				if (error) {
					console.log(`${stderr}`);
					reject(error);
					return;
				}
				console.log(`${stdout}`);
				resolve(stdout);
			});
		});
		await execPromise;
		const collection = vscode.languages.createDiagnosticCollection('test');
		if (vscode.window.activeTextEditor) {
			updateDiagnostics(vscode.window.activeTextEditor.document, collection);
		}
		vscode.window.showInformationMessage('Removed all unused includes!');
	});
	context.subscriptions.push(disposable);
}

function getRanges(): Array<vscode.Range> {
	let current_working_dir : string;
		if (vscode.window.activeTextEditor !== undefined) {
			current_working_dir = path.dirname(vscode.window.activeTextEditor.document.fileName);
		} else {
			vscode.window.showInformationMessage('Couldn\'t find ranges');
			return [];
		}
	const ranges_path = path.join(current_working_dir, "/ranges.txt");
	const ranges_file = readFileSync(ranges_path, 'utf-8');
	const lines = ranges_file.split('\n');
	let ranges = [];
	for (var line of lines) {
		const sub_range = line.split(' ');
		if (sub_range.length === 0 || sub_range.length === 1) {
			continue;
		}
		if (sub_range.length !== 3) {
			console.log("error in ranges file");
			continue;
		}
		let start_pos = new vscode.Position(Number(sub_range[0]), Number(sub_range[1]));
		let end_pos = new vscode.Position(Number(sub_range[0]), Number(sub_range[2]));
		let range = new vscode.Range(start_pos, end_pos);
		ranges.push(range);
	}
	return ranges;
}

function updateDiagnostics(document: vscode.TextDocument, collection: vscode.DiagnosticCollection): void {
	let ranges = getRanges();
	let warn_ranges = [];
	for (var i_range of ranges) {
		let warn = new vscode.Diagnostic(i_range, 'You can remove this Include', vscode.DiagnosticSeverity.Warning);
		warn_ranges.push(warn);
	}
	if (document) {
		collection.set(document.uri, warn_ranges);
	} else {
		collection.clear();
	}
}

export function deactivate() { }
