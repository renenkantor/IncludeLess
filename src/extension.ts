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
		let source_file_path = path.join(__dirname, "../src/tests/simple/main.cpp");
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
	const ranges_path = path.join(__dirname, "../src/tests/simple/ranges.txt");
	const ranges_file = readFileSync(ranges_path, 'utf-8');
	const lines = ranges_file.split('\n');
	let ranges = [];
	for (var line of lines) {
		const sub_range = line.split(' ');
		if (sub_range.length === 0) {
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
		console.log(i_range);
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
