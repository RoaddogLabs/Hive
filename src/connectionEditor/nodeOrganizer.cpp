#include "nodeorganizer.hpp"

#include <QtMate/flow/flowscene.hpp>
#include <QtMate/flow/flownode.hpp>
#include <QtMate/flow/flowinput.hpp>
#include <QtMate/flow/flowoutput.hpp>
#include <QtMate/flow/flowconnection.hpp>

namespace {

int countConnections(qtMate::flow::FlowInputs const& inputs) {
	int total = 0;
	for (auto* input : inputs) {
		total += input->isConnected();
	}
	return total;
}

int countConnections(qtMate::flow::FlowOutputs const& outputs) {
	int total = 0;
	for (auto* output : outputs) {
		total += output->connections().size();
	}
	return total;
}

// recursively traverse once, each node that is connected to the node output
using TraverseFunc = std::function<void(qtMate::flow::FlowNode*, int)>;
void traverse(qtMate::flow::FlowNode* node, int depth, TraverseFunc func) {
	func(node, depth);
	QSet<qtMate::flow::FlowNode*> visited{};
	for (auto* output : node->outputs()) {
		if (output->isConnected()) {
			for (auto* connection : output->connections()) {
				auto* input = connection->input()->node();
				if (!visited.contains(input)) {
					visited.insert(input);
					traverse(input, depth + 1, func);
				}
			}
		}
	}
}

} // namespace

NodeOrganizer::NodeOrganizer(qtMate::flow::FlowScene* scene, QObject* parent)
	: QObject{parent}
	, _scene{scene} {
	connect(_scene, &qtMate::flow::FlowScene::nodeCreated, this, [this](qtMate::flow::FlowNodeUid const& uid) {
		updateNodeData(uid);
	});

	connect(_scene, &qtMate::flow::FlowScene::nodeDestroyed, this, [this](qtMate::flow::FlowNodeUid const& uid) {
		updateNodeData(uid);
	});

	connect(_scene, &qtMate::flow::FlowScene::connectionCreated, this, [this](qtMate::flow::FlowConnectionDescriptor const& descriptor) {
		updateNodeData(descriptor.first.first);
		updateNodeData(descriptor.second.first);
	});

	connect(_scene, &qtMate::flow::FlowScene::connectionDestroyed, this, [this](qtMate::flow::FlowConnectionDescriptor const& descriptor) {
		updateNodeData(descriptor.first.first);
		updateNodeData(descriptor.second.first);
	});
}

NodeOrganizer::~NodeOrganizer() = default;

void NodeOrganizer::updateNodeData(qtMate::flow::FlowNodeUid const& uid) {
	if (auto* node = _scene->node(uid)) {
		auto& nodeData = _nodeData[uid];
		nodeData.node = node;
		nodeData.activeInputCount = countConnections(node->inputs());
		nodeData.activeOutputCount = countConnections(node->outputs());
	} else {
		delete _animations.take(uid);
		_nodeData.remove(uid);
	}

	doLayout();
}

void NodeOrganizer::doLayout() {
	using DistanceCandidates = QList<int>;
	QHash<qtMate::flow::FlowNode*, DistanceCandidates> distanceCandidates{};

	// find each node with no input (root) and compute the distance for each of its children
	for (auto const& nodeData : _nodeData) {
		// no inputs means root
		if (nodeData.activeInputCount == 0) {
			traverse(nodeData.node, 0, [&distanceCandidates](qtMate::flow::FlowNode* node, int depth) {
				distanceCandidates[node].push_front(depth);
			});
		}
	}

	// determine the actual distance for each node of the scene
	// note: distance also represents the column index in the grid
	QHash<qtMate::flow::FlowNode*, int> distances{};
	for (auto* node : _scene->nodes()) {
		if (distanceCandidates.contains(node)) {
			auto const& candidates = distanceCandidates[node];
			distances[node] = *std::max_element(std::begin(candidates), std::end(candidates));
		} else {
			distances[node] = 0;
		}
	}

	// put each node in some sort of grid
	using Column = QVector<qtMate::flow::FlowNode*>;
	using Grid = QHash<int, Column>;
	Grid grid;
	for (auto* node : _scene->nodes()) {
		auto const distance = distances.value(node, 0);
		grid[distance].emplace_back(node);
	}

	// sort each column in order to keep some consistency
	for (auto& column : grid) {
		std::sort(std::begin(column), std::end(column), [this](qtMate::flow::FlowNode* left, qtMate::flow::FlowNode* right) {
			auto const& leftData = _nodeData[left->uid()];
			auto const& rightData = _nodeData[right->uid()];

			// sort by number of connected outputs
			if (leftData.activeOutputCount != rightData.activeOutputCount) {
				return leftData.activeOutputCount > rightData.activeOutputCount;
			}

			// sort by number of connected inputs
			if (leftData.activeInputCount != rightData.activeInputCount) {
				return leftData.activeInputCount > rightData.activeInputCount;
			}

			// sort by uid
			return left->uid() < right->uid();
		});
	}

	// traverse the grid and move nodes
	auto const horizontalPadding = 120.f;
	auto const verticalPadding = 100.f;

	auto x = 0.f;
	for (auto i = 0; i < grid.size(); ++i) {
		auto const& column = grid[i];
		auto maxWidth = 0.f;

		auto y = 0.f;
		for (auto* node : column) {
			auto const r = node->boundingRect();
			if (r.width() > maxWidth) {
				maxWidth = r.width();
			}

			animateTo(node, x, y);

			y += r.height() + verticalPadding;
		}

		x += maxWidth + horizontalPadding;
	}
}

void NodeOrganizer::animateTo(qtMate::flow::FlowNode* node, float x, float y) {
	auto const startValue = node->pos();
	auto const endValue = QPointF{x, y};

	auto const uid = node->uid();
	auto* animation = _animations.value(uid);
	if (!animation) {
		animation = new QVariantAnimation{this};
		animation->setDuration(1000);
		animation->setEasingCurve(QEasingCurve::Type::OutQuart);
		_animations.insert(uid, animation);
	}

	animation->stop();
	animation->setStartValue(startValue);
	animation->setEndValue(endValue);

	// caution, pass the node uid and retrieve it in the lambda cause the node may have been deleted
	connect(animation, &QVariantAnimation::valueChanged, this, [this, uid](QVariant const& value) {
		if (auto* node = _scene->node(uid)) {
			node->setPos(value.toPointF());
		}
	});

	animation->stop();
	animation->start(QAbstractAnimation::DeletionPolicy::KeepWhenStopped);
}
