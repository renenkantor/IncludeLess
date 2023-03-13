/* eslint-disable @typescript-eslint/naming-convention */
import * as vscode from 'vscode';
import * as path from 'path';
import { readFileSync } from 'fs';
import { writeFileSync } from 'fs';
import assert = require('assert');

var exec = require('child_process').execFile;

// Called once when creating extension
export function activate(context: vscode.ExtensionContext) {
	console.log('Created IncludeLess extension.');
	const include_collection = vscode.languages.createDiagnosticCollection('include_collection');
	if (vscode.window.activeTextEditor !== undefined) {
		vscode.languages.registerCodeActionsProvider(vscode.window.activeTextEditor.document.languageId, {
			provideCodeActions(document: vscode.TextDocument, range: vscode.Range, context: vscode.CodeActionContext, token: vscode.CancellationToken): vscode.CodeAction[] | undefined {
				let fix = new vscode.CodeAction('Remove redundant include directive', vscode.CodeActionKind.QuickFix);
				fix.edit = new vscode.WorkspaceEdit();
				const lineRange = new vscode.Range(range.start.line, 0, range.start.line + 1, 0);
				fix.edit.delete(document.uri, lineRange);

				// Add a handler to the CodeAction object
				fix.command = {
					title: 'Remove include directive',
					command: 'myExtension.removeIncludeDirective',
					arguments: [document.uri, lineRange]
				};

				return [fix];
			}
		});
	}
	// Called everytime extension is used
	let disposable = vscode.commands.registerCommand('includeless.includeless', async () => {
		let source_file_path: string;
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
		if (vscode.window.activeTextEditor) {
			updateDiagnostics(vscode.window.activeTextEditor.document, include_collection);
		}
	});
	// Add a command to execute the CodeAction
	vscode.commands.registerCommand('myExtension.removeIncludeDirective', (uri: vscode.Uri, range: vscode.Range) => {
		updateRanges(path.dirname(uri.fsPath),range.start.line);
		if (vscode.window.activeTextEditor) {
			updateDiagnostics(vscode.window.activeTextEditor.document, include_collection);
		}
	});
	context.subscriptions.push(disposable);
}

function getRanges(): Array<vscode.Range> {
	let current_working_dir: string;
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

function updateRanges(current_working_dir: string, remove_line: number) {
	const ranges_path = path.join(current_working_dir, "/ranges.txt");
	const fileContent = readFileSync(ranges_path, 'utf-8');

	const lines = fileContent.split('\n');

	let remove_index: number = -1;
	for (let i = 0; i < lines.length; i++) {
		const numbers = lines[i].split(' ');
		if (parseInt(numbers[0]) > remove_line) {
			numbers[0] = (parseInt(numbers[0]) - 1).toString();
		} else if (parseInt(numbers[0]) == remove_line) {
			remove_index = i;
		}
		lines[i] = numbers.join(' ');
	}
	assert(remove_index !== -1);
	lines.splice(remove_index, 1);
	const modifiedContent = lines.join('\n');
	writeFileSync(ranges_path, modifiedContent, 'utf-8');
}

function updateDiagnostics(document: vscode.TextDocument, collection: vscode.DiagnosticCollection): void {
	collection.clear();
	let ranges = getRanges();
	let warn_ranges = [];
	for (var i_range of ranges) {
		let diagnostic = new vscode.Diagnostic(i_range, 'Redundant include directive', vscode.DiagnosticSeverity.Warning);
		diagnostic.relatedInformation = [new vscode.DiagnosticRelatedInformation(new vscode.Location(document.uri, i_range), 'This file is included but its symbols are never used')];
		warn_ranges.push(diagnostic);
	}
	if (document) {
		collection.set(document.uri, warn_ranges);
	} else {
		collection.clear();
	}
}

export function deactivate() { }
